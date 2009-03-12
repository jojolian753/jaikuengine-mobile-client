package Jaiku::BBData::SensorEvent;
use strict;
use base qw(Jaiku::BBData);
use fields qw( stamp priority data name );
use Jaiku::BBData::Simple;

sub new {
    my $class=shift;
    my $name='event';
    my $self=new Jaiku::BBData($name);

    return $class->downbless($self);
}

sub set_type_attributes {
    my $self=shift;
    $self->set_attr('{}module', '0x20006E4E');
    $self->set_attr('{}id', '10');
    $self->set_attr('{}major_version', '1');
    $self->set_attr('{}minor_version', '0');
}
sub stamp {
    my $self=shift;
    return $self->{stamp} if ($self->{stamp});
    my $ret=new Jaiku::BBData::Simple('datetime');
    $self->{stamp}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_stamp {
    my $self=shift;
    my $c=shift;
    $self->remove_child( $self->{stamp} );
    $self->{stamp}=$c;
    $self->push_child($c);
}

sub priority {
    my $self=shift;
    return $self->{priority} if ($self->{priority});
    my $ret=new Jaiku::BBData::Simple('priority');
    $self->{priority}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_priority {
    my $self=shift;
    my $c=shift;
    $self->remove_child( $self->{priority} );
    $self->{priority}=$c;
    $self->push_child($c);
}

sub name {
    my $self=shift;
    return $self->{name} if ($self->{name});
    my $ret=new Jaiku::BBData::Simple('eventname');
    $self->{name}=$ret;
    $self->push_child($ret);
    return $ret;
}

sub set_name {
    my $self=shift;
    my $c=shift;
    $self->remove_child( $self->{name} );
    $self->{name}=$c;
    $self->push_child($c);
}

sub data {
    my $self=shift;
    return $self->{data} if ($self->{data});
}

sub set_data {
    my $self=shift;
    my $c=shift;
    $self->remove_child( $self->{data} );
    $self->{data}=$c;
    $self->push_child($c);
    $c->set_type_attributes;
    $c->{element}='eventdata';
}


1;
