package Jaiku::BBData::Compound;
use strict;
use base qw(Jaiku::BBData);
use Jaiku::BBData::Factory;

sub _from_xml {
    my $self=shift;
    my $xml_to_field=$self->xml_to_field;

    my @c=@{$self->{children}};
    $self->{children}=[];

    foreach my $c (@c) {
        next unless (defined($c));
        die "non-element child $c in compound " . $self->{name} 
            unless(ref $c || $c eq "");

        if (ref $c) {
            my ($ns, $name)=$c->element;
            if ( $ns eq "" || $ns eq "http://www.cs.helsinki.fi/group/context") {
                $name = $xml_to_field->{$name};
                if ($name) {
                    my $method="set_$name";
                    $self->$method($c);
                    $c=undef;
                }
            }
        }

        $self->push_child($c) if ($c);
    }
}

sub as_parsed {
    my $self=shift;
    my $parsed={};
    my $field_to_xml=$self->field_to_xml;
    foreach my $k (keys %$field_to_xml) {
        next unless ($self->{$k});
        my $xmlname=$field_to_xml->{$k};
        $parsed->{$xmlname}=$self->{$k}->as_parsed;
    }
    return $parsed;
}

1;
