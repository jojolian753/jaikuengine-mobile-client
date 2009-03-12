package Jaiku::BBData::TupleMeta;
use strict;
use base qw(Jaiku::BBData);
use fields qw( moduleuid subname moduleid );
use Jaiku::BBData::Simple;

sub new {
    my $class=shift;
    my $name='tuplename';
    my $self=new Jaiku::BBData($name);

    return $class->downbless($self);
}

sub set_type_attributes {
    my $self=shift;
    $self->set_attr('{}module', '0x20006E4E');
    $self->set_attr('{}id', '9');
    $self->set_attr('{}major_version', '1');
    $self->set_attr('{}minor_version', '0');
}
sub moduleuid {
    my $self=shift;
    return $self->{moduleuid} if ($self->{moduleuid});
    my $ret=new Jaiku::BBData::Simple('module_uid');
    $self->{moduleuid}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_moduleuid {
    my $self=shift;
    my $c=shift;
    $self->remove_child( $self->{moduleuid} );
    $self->{moduleuid}=$c;
    $self->push_child($c);
}

sub moduleid {
    my $self=shift;
    return $self->{moduleid} if ($self->{moduleid});
    my $ret=new Jaiku::BBData::Simple('module_id');
    $self->{moduleid}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_moduleid {
    my $self=shift;
    my $c=shift;
    $self->remove_child( $self->{moduleid} );
    $self->{moduleid}=$c;
    $self->push_child($c);
}

sub subname {
    my $self=shift;
    return $self->{subname} if ($self->{subname});
    my $ret=new Jaiku::BBData::Simple('subname');
    $self->{subname}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_subname {
    my $self=shift;
    my $c=shift;
    $self->remove_child( $self->{subname} );
    $self->{subname}=$c;
    $self->push_child($c);
}


1;
