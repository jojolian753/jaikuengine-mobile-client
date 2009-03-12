package Jaiku::BBData::BuildInfo;
use strict;
use base qw(Jaiku::BBData::Compound);
use Jaiku::BBData::Base;
use fields qw( internalversion sdk buildby minorversion when majorversion branch );
use Jaiku::BBData::ShortString;
use Jaiku::BBData::Int;
use Jaiku::BBData::LongString;
use Jaiku::BBData::Time;

my $field_to_xml;
my $xml_to_field;

BEGIN {
    $field_to_xml = { 'when' => 'when', 'buildby' => 'buildby', 'sdk' => 'sdk', 'branch' => 'branch', 'majorversion' => 'majorversion', 'minorversion' => 'minorversion', 'internalversion' => 'internalversion',  };
    $xml_to_field = { 'when' => 'when', 'buildby' => 'buildby', 'sdk' => 'sdk', 'branch' => 'branch', 'majorversion' => 'majorversion', 'minorversion' => 'minorversion', 'internalversion' => 'internalversion',  };
}

sub new {
    my $class=shift;
    my $name=shift;
    my $self=new Jaiku::BBData::Compound($name);

    return $class->downbless($self);
}

sub type {
    return [ 0x20006E4E, 53, 1, 0 ];
}

sub field_to_xml {
    return $field_to_xml;
}
sub xml_to_field {
    return $xml_to_field;
}
sub when {
    my $self=shift;
    return $self->{when} if ($self->{when});
    my $ret=new Jaiku::BBData::Time('when');
    $self->{when}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_when {
    my $self=shift;
    my $c=Jaiku::BBData::Time->downbless(shift());
    $self->replace_child( $self->{when}, $c );
    $self->{when}=$c;
}

sub buildby {
    my $self=shift;
    return $self->{buildby} if ($self->{buildby});
    my $ret=new Jaiku::BBData::ShortString('buildby');
    $self->{buildby}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_buildby {
    my $self=shift;
    my $c=Jaiku::BBData::ShortString->downbless(shift());
    $self->replace_child( $self->{buildby}, $c );
    $self->{buildby}=$c;
}

sub sdk {
    my $self=shift;
    return $self->{sdk} if ($self->{sdk});
    my $ret=new Jaiku::BBData::LongString('sdk');
    $self->{sdk}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_sdk {
    my $self=shift;
    my $c=Jaiku::BBData::LongString->downbless(shift());
    $self->replace_child( $self->{sdk}, $c );
    $self->{sdk}=$c;
}

sub branch {
    my $self=shift;
    return $self->{branch} if ($self->{branch});
    my $ret=new Jaiku::BBData::LongString('branch');
    $self->{branch}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_branch {
    my $self=shift;
    my $c=Jaiku::BBData::LongString->downbless(shift());
    $self->replace_child( $self->{branch}, $c );
    $self->{branch}=$c;
}

sub majorversion {
    my $self=shift;
    return $self->{majorversion} if ($self->{majorversion});
    my $ret=new Jaiku::BBData::Int('majorversion');
    $self->{majorversion}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_majorversion {
    my $self=shift;
    my $c=Jaiku::BBData::Int->downbless(shift());
    $self->replace_child( $self->{majorversion}, $c );
    $self->{majorversion}=$c;
}

sub minorversion {
    my $self=shift;
    return $self->{minorversion} if ($self->{minorversion});
    my $ret=new Jaiku::BBData::Int('minorversion');
    $self->{minorversion}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_minorversion {
    my $self=shift;
    my $c=Jaiku::BBData::Int->downbless(shift());
    $self->replace_child( $self->{minorversion}, $c );
    $self->{minorversion}=$c;
}

sub internalversion {
    my $self=shift;
    return $self->{internalversion} if ($self->{internalversion});
    my $ret=new Jaiku::BBData::Int('internalversion');
    $self->{internalversion}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_internalversion {
    my $self=shift;
    my $c=Jaiku::BBData::Int->downbless(shift());
    $self->replace_child( $self->{internalversion}, $c );
    $self->{internalversion}=$c;
}


Jaiku::BBData::Factory::add_class();

1;
